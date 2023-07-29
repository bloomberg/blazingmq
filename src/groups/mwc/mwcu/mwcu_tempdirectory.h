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

// mwcu_tempdirectory.h                                               -*-C++-*-
#ifndef INCLUDED_MWCU_TEMPDIRECTORY
#define INCLUDED_MWCU_TEMPDIRECTORY

//@PURPOSE: Provide a guard for creating a temporary directory.
//
//@CLASSES:
//  mwcu::TempDirectory: Guard for a temporary directory
//
//@SEE_ALSO: mwcu_temputil
//
//@DESCRIPTION: This component provides a guard, 'mwcu::TempDirectory', that
// creates a directory in the effective temporary directory for the current
// process whose name is randomly assigned to guarantee no collisions with
// other directories or files in the effective temporary directory. The guarded
// directory is automatically removed when the 'mwcu::TempDirectory' object is
// destroyed.  For the definition of the effective temporary directory for the
// current process, see the mwcu_temputil component documentation.
//
/// Thread Safety
///-------------
// This component is thread safe.
//
/// Usage
///-----
// This section illustrates intended use of this component.
//
/// Usage Example 1: Creating a Temporary Directory
///- - - - - - - - - - - - - - - - - - - - -
// This example shows how to create a temporary directory which is
// automatically deleted by a 'mwcu::TempDirectory' guard.
//..
//     bsl::string dirPath;
//     {
//         mwcu::TempDirectory tempDirectory;
//         dirPath = tempDirectory.path();
//     }
//
//     BSLS_ASSERT(!bdls::FileSystemUtil::exists(dirPath));
//..

// MWC

// BDE
#include <bsl_string.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>

namespace BloombergLP {
namespace mwcu {

// ===================
// class TempDirectory
// ===================

/// This class provides a guard to create a directory in the effective
/// temporary directory for the current process that is automatically
/// removed when an object of this class is destroyed.  This class is thread
/// safe.
class TempDirectory {
    // DATA
    bsl::string d_path;

  private:
    // NOT IMPLEMENTED
    TempDirectory(const TempDirectory&);
    TempDirectory& operator=(const TempDirectory&);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(TempDirectory, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new directory in the temporary directory that is removed
    /// when this object is destroyed.  Optionally specify a
    /// `basicAllocator` used to supply memory.  If `basicAllocator` is 0,
    /// the currently installed default allocator is used.
    explicit TempDirectory(bslma::Allocator* basicAllocator = 0);

    /// Destroy this object.
    ~TempDirectory();

    // ACCESSORS

    /// Return the path to the directory.
    const bsl::string& path() const;
};

}  // close package namespace
}  // close enterprise namespace
#endif
