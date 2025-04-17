// Copyright 2014-2025 Bloomberg Finance L.P.
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
#include <m_bmqstoragetool_compositesequencenumber.h>
#include <m_bmqstoragetool_cslfileprocessor.h>
#include <m_bmqstoragetool_filters.h>
#include <m_bmqstoragetool_parameters.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

// ======================
// class CslFileProcessor
// ======================

CslFileProcessor::CslFileProcessor(
    const Parameters*                       params,
    bslma::ManagedPtr<FileManager>&         fileManager,
    const bsl::shared_ptr<CslSearchResult>& searchResult_p,
    bsl::ostream&                           ostream,
    bslma::Allocator*                       allocator)
: d_parameters(params)
, d_fileManager_mp(fileManager)
, d_ostream(ostream)
, d_searchResult_sp(searchResult_p)
, d_allocator_p(allocator)
{
    // NOTHING
}

void CslFileProcessor::process()
{
    using namespace bmqp_ctrlmsg;

    Filters filters(d_parameters->d_queueKey,
                    d_parameters->d_queueName,
                    d_parameters->d_queueMap,
                    d_parameters->d_range,
                    d_allocator_p);

    bool stopSearch = false;

    // Iterator points either to first record or to the last snapshot record
    // depending on `csl-from-begin` command line option (by default: from last
    // snapshot).
    mqbc::IncoreClusterStateLedgerIterator* iter =
        d_fileManager_mp->cslFileIterator();
    BSLS_ASSERT(iter->isValid());
    ClusterMessage clusterMessage;

    // Iterate through all cluster state ledger file records
    while (true) {
        iter->loadClusterMessage(&clusterMessage);

        mqbc::ClusterStateRecordType::Enum rt = iter->header().recordType();

        bool checkRecord = false;
        switch (rt) {
        case mqbc::ClusterStateRecordType::e_SNAPSHOT: {
            checkRecord = d_parameters->d_processCslRecordTypes.d_snapshot;
            break;  // BREAK
        }
        case mqbc::ClusterStateRecordType::e_UPDATE: {
            checkRecord = d_parameters->d_processCslRecordTypes.d_update;
            break;  // BREAK
        }
        case mqbc::ClusterStateRecordType::e_COMMIT: {
            checkRecord = d_parameters->d_processCslRecordTypes.d_commit;
            break;  // BREAK
        }
        case mqbc::ClusterStateRecordType::e_ACK: {
            checkRecord = d_parameters->d_processCslRecordTypes.d_ack;
            break;  // BREAK
        }
        default: BSLS_ASSERT(false && "Unknown record type");
        }

        if (checkRecord) {
            // Apply filters
            if (filters.apply(iter->header(),
                              clusterMessage,
                              iter->currRecordId().offset(),
                              &stopSearch)) {
                d_searchResult_sp->processRecord(iter->header(),
                                                 clusterMessage,
                                                 iter->currRecordId());
            }
        }

        // Move to the next record
        const int rc = iter->next();
        if (stopSearch || rc == 1) {
            // stopSearch is set or end iterator reached
            d_searchResult_sp->outputResult();
            return;  // RETURN
        }
        if (rc < 0) {
            d_searchResult_sp->outputResult();
            d_ostream
                << "CSL file is either corrupted or incomplete at offset="
                << iter->currRecordId().offset()
                << ". Iteration "
                   "aborted (rc="
                << rc << ").\n";
            return;  // RETURN
        }
    }
}

}  // close package namespace
}  // close enterprise namespace
