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

// bmqimp_stat.cpp                                                    -*-C++-*-
#include <bmqimp_stat.h>

#include <bmqscm_version.h>

#include <bmqst_tableutil.h>

namespace BloombergLP {
namespace bmqimp {

// -----------
// struct Stat
// -----------

Stat::Stat(bslma::Allocator* allocator)
: d_statContext_mp(0)
, d_table(allocator)
, d_tip(allocator)
, d_tableNoDelta(allocator)
, d_tipNoDelta(allocator)
{
    // NOTHING
}

void Stat::printStats(bsl::ostream& stream, bool includeDelta) const
{
    bmqst::Table* table = (includeDelta ? &d_table : &d_tableNoDelta);
    const bmqst::BasicTableInfoProvider* tip = (includeDelta ? &d_tip
                                                             : &d_tipNoDelta);

    table->records().update();
    bmqst::TableUtil::printTable(stream, *tip);
    stream << "\n";
}

// ---------------
// struct StatUtil
// ---------------

bool StatUtil::filterDirect(const bmqst::TableRecords::Record& record)
{
    return record.type() == bmqst::StatContext::e_TOTAL_VALUE;
}

bool StatUtil::filterDirectAndTopLevel(
    const bmqst::TableRecords::Record& record)
{
    return record.type() == bmqst::StatContext::e_TOTAL_VALUE &&
           record.level() != 0;
}

}  // close package namespace
}  // close enterprise namespace
