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

// mwcu_tempfile.h                                                    -*-C++-*-
#ifndef INCLUDED_MWCU_TEMPFILE
#define INCLUDED_MWCU_TEMPFILE

//@PURPOSE: Provide a guard for creating a temporary file.
//
//@CLASSES:
//  mwcu::TempFile: Guard for a temporary file
//
//@SEE_ALSO: mwcu_temputil
//
//@DESCRIPTION: This component provides a guard, 'mwcu::TempFile', that creates
// a file in the effective temporary directory for the current process whose
// name is randomly assigned to guarantee no collisions with other directories
// or files in the effective temporary directory. The guarded file is
// automatically removed when the 'mwcu::TempFile' object is destroyed.  For
// the definition of the effective temporary directory for the current process,
// see the mwcu_temputil component documentation.
//
/// Thread Safety
///-------------
// This component is thread safe.
//
/// Usage
///-----
// This section illustrates intended use of this component.
//
/// Usage Example 1: Creating a Temporary File
///- - - - - - - - - - - - - - - - - - - - -
// This example shows how to create a temporary file which is automatically
// deleted by a 'mwcu::TemporaryFile' guard.
//..
//     bsl::string filePath;
//     {
//         mwcu::TempFile tempFile;
//         filePath = tempFile.path();
//
//         bsl::ofstream ofs(filePath.c_str());
//         BSLS_ASSERT(ofs);
//
//         ofs << "Hello, world!";
//     }
//
//     BSLS_ASSERT(!bdls::FileSystemUtil::exists(filePath));
//..

// MWC

// BDE
#include <bsl_string.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>

namespace BloombergLP {
namespace mwcu {

// ==============
// class TempFile
// ==============

/// This class provides a guard to create a file in the effective temporary
/// directory for the current process that is automatically removed when an
/// object of this class is destroyed.  This class is thread safe.
class TempFile {
    // DATA
    bsl::string d_path;

  private:
    // NOT IMPLEMENTED
    TempFile(const TempFile&);
    TempFile& operator=(const TempFile&);

  public:
    // TRAITS
    BSLMF_NESTED_TRAIT_DECLARATION(TempFile, bslma::UsesBslmaAllocator)

    // CREATORS

    /// Create a new file in the temporary directory that is removed when
    /// this object is destroyed.  Optionally specify a `basicAllocator`
    /// used to supply memory.  If `basicAllocator` is 0, the currently
    /// installed default allocator is used.
    explicit TempFile(bslma::Allocator* basicAllocator = 0);

    /// Destroy this object.
    ~TempFile();

    // ACCESSORS

    /// Return the path to the file.
    const bsl::string& path() const;
};

}  // close package namespace
}  // close enterprise namespace
#endif
