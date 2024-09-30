# Copyright 2024 Bloomberg Finance L.P.
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""
Script for checking PR title.
The script expects PR_TITLE environment variable to be set as an input parameter.
Usage:
export PR_TITLE="<...>"
python3 check_pr_title.py
"""
import os


def check_pr_title():
    title = os.environ.get("PR_TITLE")
    if title is None or len(title) == 0:
        raise RuntimeError(
            "This script expects a non-empty environment variable PR_TITLE set"
        )
    title = title.lower()
    valid_prefixes = [
        "fix",
        "feat",
        "perf",
        "ci",
        "build",
        "revert",
        "ut",
        "it",
        "doc",
        "refactor",
        "misc",
        "test",
    ]
    if not any(title.startswith(prefix.lower()) for prefix in valid_prefixes):
        raise RuntimeError(
            "PR title \"{}\" doesn't start with a valid prefix, allowed prefixes: {}".format(
                title, " ".join(valid_prefixes)
            )
        )


if __name__ == "__main__":
    check_pr_title()
