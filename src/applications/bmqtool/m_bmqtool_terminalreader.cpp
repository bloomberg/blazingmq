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

#include <m_bmqtool_terminalreader.h>

// BDE
#include <bdlb_string.h>
#include <bsl_iostream.h>
#include <bslma_default.h>
#include <bsls_assert.h>

// SYSTEM
#include <termios.h>
#include <unistd.h>

namespace BloombergLP {
namespace m_bmqtool {

namespace {

// ANSI escape sequences for terminal control
const bsl::string_view k_CLEAR_TO_END_OF_LINE = "\x1b[K";
const bsl::string_view k_CLEAR_SCREEN         = "\x1b[2J\x1b[H";

/// Control Sequence Introducer
const bsl::string_view k_CSI = "\x1b[";

// ==================
// class RawModeGuard
// ==================

/// @brief RAII guard that switches stdin to raw mode on construction and
/// restores the original terminal settings on destruction.
class RawModeGuard {
    // NOT IMPLEMENTED
    RawModeGuard(const RawModeGuard&);             // = delete
    RawModeGuard& operator=(const RawModeGuard&);  // = delete

    // DATA
    struct termios d_orig;    ///< Original terminal settings.
    bool           d_active;  ///< Whether raw mode was successfully applied.

  public:
    // CREATORS

    /// @brief Create a guard that switches stdin to raw mode.
    ///
    /// If the terminal attributes cannot be read or set, the guard is
    /// inactive ('isActive' returns 'false') and the destructor is a
    /// no-op.
    RawModeGuard()
    : d_active(false)
    {
        // Save the current terminal attributes.  'tcgetattr' fails when
        // stdin is not a real terminal (pipe, redirect), in which case we
        // leave 'd_active' false and skip raw-mode setup entirely.
        if (tcgetattr(STDIN_FILENO, &d_orig) != 0) {
            return;  // RETURN
        }

        struct termios raw = d_orig;

        // ICANON: disable canonical (line-buffered) mode so we receive
        //         each keypress immediately.
        // ECHO:   disable automatic character echo so we control what is
        //         printed to the terminal.
        // ISIG:   disable signal generation from Ctrl-C / Ctrl-Z so we
        //         can handle these keys ourselves.
        raw.c_lflag &= ~(ICANON | ECHO | ISIG);

        // VMIN = 1, VTIME = 0: blocking read that returns as soon as at
        // least 1 byte is available.
        raw.c_cc[VMIN]  = 1;
        raw.c_cc[VTIME] = 0;

        // TCSAFLUSH: apply the new settings after all pending output has
        // been written and discard any unread input.
        if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == 0) {
            d_active = true;
        }
    }

    /// @brief Restore the original terminal settings.
    ~RawModeGuard()
    {
        if (d_active) {
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &d_orig);
        }
    }

    // ACCESSORS

    /// @brief Return 'true' if raw mode is active.
    /// @return 'true' if raw mode was successfully applied
    bool isActive() const { return d_active; }
};

/// @brief Clear and redraw the current terminal line.
///
/// @param prompt the prompt string displayed before the input
/// @param line   the current input line content
/// @param cursor the cursor position within @p line (0-based)
void refreshLine(bsl::string_view prompt, const bsl::string& line, int cursor)
{
    // Move to column 0, clear from cursor to end of line, reprint prompt +
    // line, then reposition cursor
    bsl::cout << "\r" << k_CLEAR_TO_END_OF_LINE << prompt << line;

    // Move cursor to correct position
    int promptLen = static_cast<int>(prompt.length());
    int totalLen  = promptLen + static_cast<int>(line.size());
    int target    = promptLen + cursor;
    if (target < totalLen) {
        if (target > 0) {
            bsl::cout << "\r" << k_CSI << target << "C";
        }
        else {
            bsl::cout << "\r";
        }
    }
    bsl::cout << bsl::flush;
}

}  // close unnamed namespace

// --------------------
// class TerminalReader
// --------------------

// CREATORS
TerminalReader::TerminalReader(bslma::Allocator* allocator)
: d_allocator_p(bslma::Default::allocator(allocator))
, d_history(d_allocator_p)
{
}

// MANIPULATORS
bool TerminalReader::getLine(bsl::string* out, bsl::string_view prompt)
{
    // PRECONDITIONS
    BSLS_ASSERT(out);

    if (!isatty(STDIN_FILENO)) {
        // Non-TTY fallback (piped input)
        bsl::cout << prompt << bsl::flush;
        bsl::cin.clear();
        if (!bsl::getline(bsl::cin, *out)) {
            bsl::cout << bsl::endl;
            return false;  // RETURN
        }
        bdlb::String::trim(out);
        return true;  // RETURN
    }

    RawModeGuard guard;
    if (!guard.isActive()) {
        // Fallback: raw mode failed, use standard getline
        bsl::cout << prompt << bsl::flush;
        if (!bsl::getline(bsl::cin, *out)) {
            return false;  // RETURN
        }
        bdlb::String::trim(out);
        return true;  // RETURN
    }

    bsl::cout << prompt << bsl::flush;

    bsl::string line(d_allocator_p);
    int         cursor     = 0;
    int         historyIdx = static_cast<int>(d_history.size());
    bsl::string savedLine(d_allocator_p);  // saves current line when
                                           // navigating history

    while (true) {
        char c;
        if (read(STDIN_FILENO, &c, 1) != 1) {
            // Read error or EOF
            bsl::cout << bsl::endl;
            return false;  // RETURN
        }

        if (c == '\r' || c == '\n') {
            // Enter: submit line
            bsl::cout << bsl::endl;
            bdlb::String::trim(&line);
            *out = line;
            if (!out->empty()) {
                if (d_history.empty() || d_history.back() != line) {
                    d_history.push_back(line);
                }
            }
            return true;  // RETURN
        }

        if (c == 4) {
            // Ctrl-D
            if (line.empty()) {
                bsl::cout << bsl::endl;
                return false;  // RETURN
            }
            // Non-empty line: delete char under cursor (like forward-delete)
            if (cursor < static_cast<int>(line.size())) {
                line.erase(cursor, 1);
                refreshLine(prompt, line, cursor);
            }
            continue;  // CONTINUE
        }

        if (c == 3) {
            // Ctrl-C: clear current line
            line.clear();
            cursor = 0;
            bsl::cout << "\n" << prompt << bsl::flush;
            continue;  // CONTINUE
        }

        if (c == 1) {
            // Ctrl-A: home
            cursor = 0;
            refreshLine(prompt, line, cursor);
            continue;  // CONTINUE
        }

        if (c == 5) {
            // Ctrl-E: end
            cursor = static_cast<int>(line.size());
            refreshLine(prompt, line, cursor);
            continue;  // CONTINUE
        }

        if (c == 127 || c == 8) {
            // Backspace
            if (cursor > 0) {
                line.erase(cursor - 1, 1);
                --cursor;
                refreshLine(prompt, line, cursor);
            }
            continue;  // CONTINUE
        }

        if (c == 12) {
            // Ctrl-L: clear screen and redraw
            bsl::cout << k_CLEAR_SCREEN;
            refreshLine(prompt, line, cursor);
            continue;  // CONTINUE
        }

        if (c == '\x1b') {
            // Escape sequence
            char seq[2];
            if (read(STDIN_FILENO, &seq[0], 1) != 1) {
                continue;  // CONTINUE
            }
            if (read(STDIN_FILENO, &seq[1], 1) != 1) {
                continue;  // CONTINUE
            }

            if (seq[0] == '[') {
                switch (seq[1]) {
                case 'A': {
                    // Up arrow: previous history
                    if (historyIdx > 0) {
                        if (historyIdx == static_cast<int>(d_history.size())) {
                            savedLine = line;
                        }
                        --historyIdx;
                        line   = d_history[historyIdx];
                        cursor = static_cast<int>(line.size());
                        refreshLine(prompt, line, cursor);
                    }
                } break;

                case 'B': {
                    // Down arrow: next history
                    if (historyIdx < static_cast<int>(d_history.size())) {
                        ++historyIdx;
                        if (historyIdx == static_cast<int>(d_history.size())) {
                            line = savedLine;
                        }
                        else {
                            line = d_history[historyIdx];
                        }
                        cursor = static_cast<int>(line.size());
                        refreshLine(prompt, line, cursor);
                    }
                } break;

                case 'C': {
                    // Right arrow
                    if (cursor < static_cast<int>(line.size())) {
                        ++cursor;
                        refreshLine(prompt, line, cursor);
                    }
                } break;

                case 'D': {
                    // Left arrow
                    if (cursor > 0) {
                        --cursor;
                        refreshLine(prompt, line, cursor);
                    }
                } break;

                case 'H': {
                    // Home
                    cursor = 0;
                    refreshLine(prompt, line, cursor);
                } break;

                case 'F': {
                    // End
                    cursor = static_cast<int>(line.size());
                    refreshLine(prompt, line, cursor);
                } break;

                case '3': {
                    // Delete key: \x1b[3~
                    char tilde;
                    if (read(STDIN_FILENO, &tilde, 1) == 1 && tilde == '~') {
                        if (cursor < static_cast<int>(line.size())) {
                            line.erase(cursor, 1);
                            refreshLine(prompt, line, cursor);
                        }
                    }
                } break;

                default: {
                    break;
                }
                }
            }
            continue;  // CONTINUE
        }

        if (static_cast<unsigned char>(c) >= 32) {
            // Printable character
            line.insert(line.begin() + cursor, c);
            ++cursor;
            refreshLine(prompt, line, cursor);
        }
    }
}

}  // close package namespace
}  // close enterprise namespace
