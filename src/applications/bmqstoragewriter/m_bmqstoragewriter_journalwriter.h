// Copyright 2024 Bloomberg Finance L.P.
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

#ifndef INCLUDED_M_BMQSTORAGEWRITER_JOURNALWRITER
#define INCLUDED_M_BMQSTORAGEWRITER_JOURNALWRITER

#include <m_bmqstoragewriter_cslwriter.h>

#include <bdls_filesystemutil.h>
#include <bsl_iosfwd.h>
#include <bslma_allocator.h>

namespace BloombergLP {
namespace m_bmqstoragewriter {

int processJournalInput(const QueueCache&                    cache,
                        bdls::FilesystemUtil::FileDescriptor journalFd,
                        bdls::FilesystemUtil::FileDescriptor dataFd,
                        bdls::FilesystemUtil::FileDescriptor qlistFd,
                        bsl::istream&                        input,
                        bslma::Allocator*                    allocator);

}  // close package namespace
}  // close enterprise namespace

#endif
