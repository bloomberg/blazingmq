# Copyright 2025 Bloomberg Finance L.P.
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

# This file contains implentation related to checked test result
# Filled via pytest hooks in 'conftest.py'
# Used in 'fixtures.py'

from typing import Dict
from pytest import StashKey, CollectReport

PHASE_REPORT_KEY = StashKey[Dict[str, CollectReport]]()


def is_test_reported_failed(request):
    # Use the test results reported by the pytest_runtest_makereport hook
    # To decide if test failed

    report = request.node.stash[PHASE_REPORT_KEY]
    if report["setup"].failed:
        return True
    if report["setup"].skipped:
        return True
    if ("call" not in report) or report["call"].failed:
        return True

    return False
