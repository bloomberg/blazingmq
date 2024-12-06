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
#include "m_bmqstoragetool_compositesequencenumber.h"
#include "m_bmqstoragetool_parameters.h"
#include <m_bmqstoragetool_cslfileprocessor.h>
#include <m_bmqstoragetool_filters.h>

// MQB
#include <mqbs_filestoreprotocolprinter.h>
#include <mqbs_filestoreprotocolutil.h>
#include <mqbs_filesystemutil.h>
#include <mqbs_offsetptr.h>

// BMQ
#include <bmqu_alignedprinter.h>
#include <bmqu_memoutstream.h>
#include <bmqu_outstreamformatsaver.h>
#include <bmqu_stringutil.h>

// BDE
#include <bdls_filesystemutil.h>
#include <bsl_iostream.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace m_bmqstoragetool {

// ======================
// class CslFileProcessor
// ======================

CslFileProcessor::CslFileProcessor(
    const Parameters*               params,
    bslma::ManagedPtr<FileManager>& fileManager,
    // const bsl::shared_ptr<SearchResult>& searchResult_p,
    bsl::ostream&     ostream,
    bslma::Allocator* allocator)
: d_parameters(params)
, d_fileManager(fileManager)
, d_ostream(ostream)
// , d_searchResult_p(searchResult_p)
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
    // bool needMoveToLowerBound = d_parameters->d_range.d_timestampGt > 0 ||
    //                             d_parameters->d_range.d_offsetGt > 0 ||
    //                             d_parameters->d_range.d_seqNumGt.isSet();

    // Iterator points either to first record or to the last snapshot record
    // depending on `parameters->d_cslFromBegin`
    mqbc::IncoreClusterStateLedgerIterator* iter =
        d_fileManager->cslFileIterator();
    BSLS_ASSERT(iter->isValid());
    ClusterMessage clusterMessage;

    // Iterate through all cluster state ledger file records
    while (true) {
        iter->loadClusterMessage(&clusterMessage);

        if (iter->header().recordType() ==
            mqbc::ClusterStateRecordType::e_SNAPSHOT) {
            if (d_parameters->d_processCslRecordTypes.d_snapshot) {
                d_ostream << *iter << '\n';
                // d_ostream << iter->header() << '\n';
                d_ostream << clusterMessage << '\n';
            }
        }
        else if (iter->header().recordType() ==
                 mqbc::ClusterStateRecordType::e_UPDATE) {
            if (d_parameters->d_processCslRecordTypes.d_update) {
                d_ostream << *iter << '\n';
                // d_ostream << iter->header() << '\n';
                d_ostream << clusterMessage << '\n';
            }
        }
        else if (iter->header().recordType() ==
                 mqbc::ClusterStateRecordType::e_COMMIT) {
            if (d_parameters->d_processCslRecordTypes.d_commit) {
                d_ostream << *iter << '\n';
                // d_ostream << iter->header() << '\n';
                d_ostream << clusterMessage << '\n';
            }
        }
        else if (iter->header().recordType() ==
                 mqbc::ClusterStateRecordType::e_ACK) {
            if (d_parameters->d_processCslRecordTypes.d_ack) {
                d_ostream << *iter << '\n';
                // d_ostream << iter->header() << '\n';
                d_ostream << clusterMessage << '\n';
            }
        }
        else {
            BSLS_ASSERT(false && "Unknown record type");
        }

        // Move to the next record
        const int rc = iter->next();
        if (stopSearch || rc == 1) {
            // stopSearch is set or end iterator reached
            // d_searchResult_p->outputResult();
            return;  // RETURN
        }
        if (rc < 0) {
            d_ostream << "CSL file is corrupted or incomplete. Iteration "
                         "aborted (rc="
                      << rc << ").";
            return;  // RETURN
        }
    }
}

}  // close package namespace
}  // close enterprise namespace
