// Copyright 2026 Bloomberg Finance L.P.
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

#ifndef INCLUDED_BMQU_TODO
#define INCLUDED_BMQU_TODO

///@PURPOSE: This component provides a utility macro for indicating stubs in
/// impelementations.
///
///@CLASSES:
///   bmqu::Todo: An object which throws on conversion to a return type. Not
///   meant to be used directly.
///
///@MACROS:
///   BMQU_TODO(...): A macro used to indicate more implementation is
///   necessary, but allows the program to compile. Optionally provide a
///   message for printing to add additional context.
///
///@DESCRIPTION:
/// This is a simple component that is meant for use during development. Say
/// you are working on a function `void foo()` that you wish to stub out before
/// implementing (possibly because you are writing other interfaces or tests
/// first):
///
/// ```c++
/// void foo()
/// {
///     BMQU_TODO();
/// }
/// ```
///
/// `BMQU_TODO()` will correctly evaluate to a type that is valid in the
/// context of whatever place it is being called in, which means it will always
/// ensure your program compiles in contexts where you might not have a valid
/// value of the type you need and it is hard to create. Additionally, in the
/// spirit of being explicit it will help remove ambiguity about whether or not
/// a placeholder value is a placeholder.
///
/// If a function is ever called that evaluates `BMQU_TODO()`, the program will
/// immediately exit and print a filename and line number where the `BMQU_TODO`
/// expression was evaluated. It is also possible to provide an additional
/// message for extra context.
///
/// ```c++
/// bsl::shared_ptr<UnconstructibleType> doSomeWork()
/// {
///     if (needToDoWork())
///     {
///         return BMQU_TODO("Work needs to be done");
///     }
///     else
///     {
///         BALL_LOG_INFO << "No work to do";
///         return NULL;
///     }
/// }
/// ```

// BDE
#include <bsl_format.h>
#include <bsls_assert.h>

namespace BloombergLP {
namespace bmqu {

class Todo {
  private:
    const char* d_message;

  public:
    Todo();
    explicit Todo(const char* message);

    template <typename T>
    operator T();
};

// INLINE DEFINITIONS

inline Todo::Todo()
: d_message(NULL)
{
}

inline Todo::Todo(const char* message)
: d_message(message)
{
}

template <typename T>
inline Todo::operator T()
{
    bsl::string message;
    if (d_message == NULL) {
        message = bsl::format("not yet implemented: {}", d_message);
    }
    else {
        message = bsl::format("not yet implemented");
    }
    BSLS_ASSERT_INVOKE_NORETURN(message.c_str());
}

}
}

#define BMQU_TODO(...) (BloombergLP::bmqu::Todo(__VA_ARGS__))

#endif
