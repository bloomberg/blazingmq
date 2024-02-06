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

// bmqstoragetool
#include <m_bmqstoragetool_commandprocessorfactory.h>
#include <m_bmqstoragetool_searchresultfactory.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

// =============================
// class CommandProcessorFactory
// =============================

bsl::unique_ptr<CommandProcessor>
CommandProcessorFactory::createCommandProcessor(
    bsl::unique_ptr<Parameters> params,
    bsl::ostream&               ostream,
    bslma::Allocator*           allocator)
{
    // Create searchResult for given 'params'.
    bsl::shared_ptr<SearchResult> searchResult =
        SearchResultFactory::createSearchResult(params.get(),
                                                ostream,
                                                allocator);
    // Create commandProcessor.
    bsl::unique_ptr<CommandProcessor> commandProcessor;
    commandProcessor = bsl::make_unique<JournalFileProcessor>(
        bsl::move(params),
        ostream,
        searchResult,
        allocator);
    return commandProcessor;
}

}  // close package namespace
}  // close enterprise namespace
