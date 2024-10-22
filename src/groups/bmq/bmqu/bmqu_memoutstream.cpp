// Copyright 2017-2023 Bloomberg Finance L.P.
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

// bmqu_memoutstream.cpp                                              -*-C++-*-
#include <bmqu_memoutstream.h>

#include <bmqscm_version.h>

namespace BloombergLP {
namespace bmqu {

MemOutStream::~MemOutStream()
{
    // This empty implementation is here because certain compilers need at
    // least one virtual member function to be defined in the implementation
    // file so they know where they should put the vtable.
}

}  // close package namespace
}  // close enterprise namespace
