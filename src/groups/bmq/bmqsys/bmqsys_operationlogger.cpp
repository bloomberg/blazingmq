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

// bmqsys_operationlogger.cpp                                         -*-C++-*-
#include <bmqsys_operationlogger.h>

#include <bmqscm_version.h>

namespace BloombergLP {
namespace bmqsys {

// ---------------------
// class OperationLogger
// ---------------------

OperationLogger::OperationLogger(bslma::Allocator* allocator)
: d_beginTimestamp(0)
, d_currentOpDescription(allocator)
{
    // NOTHING
}

OperationLogger::~OperationLogger()
{
    if (d_beginTimestamp != 0) {
        stop();
    }
}

}  // close package namespace
}  // close enterprise namespace
