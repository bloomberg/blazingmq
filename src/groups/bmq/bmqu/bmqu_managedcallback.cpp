// Copyright 2025 Bloomberg Finance L.P.
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

// bmqu_managedcallback.cpp                                           -*-C++-*-
#include <bmqu_managedcallback.h>

#include <bmqscm_version.h>

namespace BloombergLP {
namespace bmqu {

namespace {

class VoidCallback : public bmqu::ManagedCallback::CallbackFunctor {
  private:
    // PRIVATE DATA
    bmqu::ManagedCallback::VoidFunctor d_callback;

  public:
    // CREATORS
    explicit VoidCallback(const bmqu::ManagedCallback::VoidFunctor& callback)
    : d_callback(callback)
    {
        // NOTHING
    }

    explicit VoidCallback(
        bslmf::MovableRef<bmqu::ManagedCallback::VoidFunctor> callback)
    : d_callback(bslmf::MovableRefUtil::move(callback))
    {
        // NOTHING
    }

    ~VoidCallback() BSLS_KEYWORD_OVERRIDE
    {
        // NOTHING
    }

    // ACCESSORS
    void operator()() const BSLS_KEYWORD_OVERRIDE
    {
        if (d_callback) {
            d_callback();
        }
    }
};

}  // close unnamed namespace

// ---------------------------------------
// struct ManagedCallback::CallbackFunctor
// ---------------------------------------

ManagedCallback::CallbackFunctor::~CallbackFunctor()
{
    // NOTHING
}

// ----------------------
// struct ManagedCallback
// ----------------------

void ManagedCallback::set(const VoidFunctor& callback)
{
    // Preconditions for placement are checked in `place`.
    // Destructor is called by `reset` of the holding DispatcherEvent.
    new (place<VoidCallback>()) VoidCallback(callback);
}

void ManagedCallback::set(bslmf::MovableRef<VoidFunctor> callback)
{
    // Preconditions for placement are checked in `place`.
    // Destructor is called by `reset` of the holding DispatcherEvent.
    new (place<VoidCallback>())
        VoidCallback(bslmf::MovableRefUtil::move(callback));
}

}  // close package namespace
}  // close enterprise namespace
