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

#include <bmqp_event.h>

#include <bmqp_ackmessageiterator.h>
#include <bmqp_confirmmessageiterator.h>
#include <bmqp_pushmessageiterator.h>
#include <bmqp_putmessageiterator.h>
#include <bmqp_recoverymessageiterator.h>
#include <bmqp_rejectmessageiterator.h>
#include <bmqp_storagemessageiterator.h>

#include <bmqscm_version.h>
// BDE
#include <bsl_iostream.h>
#include <bslim_printer.h>

namespace BloombergLP {
namespace bmqp {

// -----------
// class Event
// -----------

void Event::loadAckMessageIterator(AckMessageIterator* iterator) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isAckEvent());
    BSLS_ASSERT_SAFE(isValid());

    iterator->reset(blob(), *d_header);
}

void Event::loadConfirmMessageIterator(ConfirmMessageIterator* iterator) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isConfirmEvent());
    BSLS_ASSERT_SAFE(isValid());

    iterator->reset(blob(), *d_header);
}

void Event::loadPushMessageIterator(PushMessageIterator* iterator,
                                    bool                 decompressFlag) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isPushEvent());
    BSLS_ASSERT_SAFE(isValid());

    iterator->reset(blob(), *d_header, decompressFlag);
}

void Event::loadPutMessageIterator(PutMessageIterator* iterator,
                                   bool                decompressFlag) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isPutEvent());
    BSLS_ASSERT_SAFE(isValid());

    iterator->reset(blob(), *d_header, decompressFlag);
}

void Event::loadStorageMessageIterator(StorageMessageIterator* iterator) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isStorageEvent() || isPartitionSyncEvent());
    BSLS_ASSERT_SAFE(isValid());

    iterator->reset(blob(), *d_header);
}

void Event::loadRecoveryMessageIterator(
    RecoveryMessageIterator* iterator) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isRecoveryEvent());
    BSLS_ASSERT_SAFE(isValid());

    iterator->reset(blob(), *d_header);
}

void Event::loadRejectMessageIterator(RejectMessageIterator* iterator) const
{
    // PRECONDITIONS
    BSLS_ASSERT_SAFE(isRejectEvent());
    BSLS_ASSERT_SAFE(isValid());

    iterator->reset(blob(), *d_header);
}

bsl::ostream&
Event::print(bsl::ostream& stream, int level, int spacesPerLevel) const
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("type", type());
    printer.end();

    return stream;
}

}  // close package namespace
}  // close enterprise namespace
