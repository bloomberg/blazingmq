// Copyright 2023 Bloomberg Finance L.P.
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

/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 Krzysztof Narkiewicz <krzysztof.narkiewicz@ezaquarii.com>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef INCLUDED_BMQEVAL_SIMPLEEVALUATORSCANNER_DEFINED
#define INCLUDED_BMQEVAL_SIMPLEEVALUATORSCANNER_DEFINED

/**
 * Generated Flex class name is yyFlexLexer by default. If we want to use more
 * flex-generated classes we should name them differently. See scanner.l prefix
 * option.
 *
 * Unfortunately the implementation relies on this trick with redefining class
 * name with a preprocessor macro. See GNU Flex manual, "Generating C++
 * Scanners" section
 */
#if !defined(yyFlexLexerOnce)
#undef yyFlexLexer
#define yyFlexLexer                                                           \
    BloombergLP_bmqeval_simpleevaluator_FlexLexer  // the trick with prefix; no
                                                   // namespace here :(
#include <FlexLexer.h>
#endif

// Scanner method signature is defined by this macro. Original yylex() returns
// int.  Sinice Bison 3 uses symbol_type, we must change returned type. We also
// rename it to something sane, since you cannot overload return type.
#undef YY_DECL
#define YY_DECL                                                               \
    SimpleEvaluatorParser::symbol_type SimpleEvaluatorScanner::get_next_token()

#define register
// We compile in C++17, and 'register' is not allowed as a storage class
// anymore, so disable the keyword. If this is too controversial, we can write
// the scanner by hand.

#include <bmqeval_simpleevaluatorparser.hpp>  // this is needed for symbol_type

namespace BloombergLP {
namespace bmqeval {

class SimpleEvaluatorScanner : public yyFlexLexer {
  public:
    SimpleEvaluatorScanner()
    : d_location(0)
    , d_lastTokenLength(0)
    {
    }

    virtual SimpleEvaluatorParser::symbol_type get_next_token();

    size_t d_location;
    size_t d_lastTokenLength;

    void updatePosition()
    {
        d_lastTokenLength = strlen(yytext);
        d_location += d_lastTokenLength;
    }

    size_t lastTokenLocation() const { return d_location - d_lastTokenLength; }
};

}
}

#endif
