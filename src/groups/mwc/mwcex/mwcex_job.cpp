// Copyright 2021-2023 Bloomberg Finance L.P.
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

// mwcex_job.cpp                                                      -*-C++-*-
#include <mwcex_job.h>

#include <mwcscm_version.h>
namespace BloombergLP {
namespace mwcex {

// --------------------
// class Job_TargetBase
// --------------------

Job_TargetBase::~Job_TargetBase()
{
    // NOTHING
}

// ---------
// class Job
// ---------

// CREATORS
Job::~Job()
{
    d_target.deleteObject<Job_TargetBase>();
}

// MANIPULATORS
void Job::operator()()
{
    d_target.object<Job_TargetBase>()->invoke();
}

}  // close package namespace
}  // close enterprise namespace
