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

import subprocess
import re


def test_breathing(storagetool):
    """
    This test checks that storage tool could be run without arguments and print error.
    """
    res = subprocess.run([storagetool], capture_output=True)
    assert res.returncode == 255
    assert (
        re.search(b"Neither journal path nor journal file are specified", res.stderr)
        is not None
    )


def test_print_help(storagetool):
    """
    This test checks that storage tool could print help info.
    """

    res = subprocess.run([storagetool, "-h"], capture_output=True)
    assert res.returncode == 255
    assert re.search(r"--help\s+print usage", res.stderr.decode()) is not None


def test_wrong_argument(storagetool):
    """
    This test checks that storage tool prints error message if wrong argument passed.
    """
    res = subprocess.run([storagetool, "--foo"], capture_output=True)
    assert res.returncode == 255
    assert (
        re.search(
            r"The string \"foo\" does not match any long tag.",
            res.stderr.decode("utf-8"),
        )
        is not None
    )
