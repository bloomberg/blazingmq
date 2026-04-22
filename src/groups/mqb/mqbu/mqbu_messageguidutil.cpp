// Copyright 2015-2023 Bloomberg Finance L.P.
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

// mqbu_messageguidutil.cpp                                           -*-C++-*-
#include <mqbu_messageguidutil.h>

#include <mqbscm_version.h>

// BMQ
#include <bmqio_resolveutil.h>
#include <bmqp_messageguidgenerator.h>
#include <bmqu_memoutstream.h>

// BDE
#include <bdlb_bigendian.h>
#include <bdlb_bitmaskutil.h>
#include <bdlb_print.h>
#include <bdlde_md5.h>
#include <bdlma_localsequentialallocator.h>
#include <bdls_processutil.h>
#include <bsl_cstring.h>
#include <bsl_iostream.h>
#include <bsl_limits.h>
#include <bsl_string.h>
#include <bslmt_once.h>
#include <bsls_assert.h>
#include <bsls_atomic.h>
#include <bsls_systemtime.h>
#include <bsls_timeutil.h>
#include <bsls_types.h>

// NTC
#include <ntsa_error.h>
#include <ntsa_ipv4address.h>

namespace BloombergLP {
namespace mqbu {

namespace {  // unnamed namespace

bmqp::MessageGUIDGenerator& guidGenerator()
{
    static bmqp::MessageGUIDGenerator* s_guidGenerator_p = NULL;
    BSLMT_ONCE_DO
    {
        static bmqp::MessageGUIDGenerator s_guidGenerator(0);
        s_guidGenerator_p = &s_guidGenerator;
    }
    return *s_guidGenerator_p;
}

}  // close unnamed namespace

// ---------------------
// class MessageGUIDUtil
// ---------------------

// CLASS LEVEL METHODS
void MessageGUIDUtil::initialize()
{
    // NOTHING
    (void)guidGenerator();
}

void MessageGUIDUtil::generateGUID(bmqt::MessageGUID* guid)
{
    guidGenerator().generateGUID(guid);
}

const char* MessageGUIDUtil::brokerIdHex()
{
    return guidGenerator().clientIdHex();
}

void MessageGUIDUtil::extractFields(int*                     version,
                                    unsigned int*            counter,
                                    bsls::Types::Int64*      timerTick,
                                    bsl::string*             brokerId,
                                    const bmqt::MessageGUID& guid)
{
    guidGenerator().extractFields(version, counter, timerTick, brokerId, guid);
}

bsl::ostream& MessageGUIDUtil::print(bsl::ostream&            stream,
                                     const bmqt::MessageGUID& guid)
{
    if (stream.bad()) {
        return stream;  // RETURN
    }

    if (guid.isUnset()) {
        stream << "** UNSET **";
        return stream;  // RETURN
    }

    int                version;
    unsigned int       counter;
    bsls::Types::Int64 timerTick;
    bsl::string        brokerId;

    extractFields(&version, &counter, &timerTick, &brokerId, guid);
    stream << "[version: " << version << ", "
           << "counter: " << counter << ", "
           << "timerTick: " << timerTick << ", "
           << "brokerId: " << brokerId << "]";

    return stream;
}

}  // close package namespace
}  // close enterprise namespace
