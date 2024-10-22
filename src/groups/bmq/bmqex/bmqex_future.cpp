// Copyright 2018-2023 Bloomberg Finance L.P.
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

// bmqex_future.cpp                                                   -*-C++-*-
#include <bmqex_future.h>

#include <bmqscm_version.h>

namespace BloombergLP {
namespace bmqex {

// ----------------------
// class Future_Exception
// ----------------------

// CREATORS
Future_Exception::~Future_Exception()
{
    d_target.deleteObject<TargetBase>();
}

// ACCESSORS
void Future_Exception::emit() const
{
    d_target.object<TargetBase>()->emit();  // does throw
}

// ----------------------------------
// class Future_Exception::TargetBase
// ----------------------------------

// CREATORS
Future_Exception::TargetBase::~TargetBase()
{
    // NOTHING
}

// ---------------------
// class Future_Callback
// ---------------------

// CREATORS
Future_Callback::~Future_Callback()
{
    d_target.deleteObject<TargetBase>();
}

// MANIPULATORS
void Future_Callback::invoke(void* sharedState)
{
    d_target.object<TargetBase>()->invoke(sharedState);
}

// ---------------------------------
// class Future_Callback::TargetBase
// ---------------------------------

// CREATORS
Future_Callback::TargetBase::~TargetBase()
{
    // NOTHING
}

}  // close package namespace
}  // close enterprise namespace
