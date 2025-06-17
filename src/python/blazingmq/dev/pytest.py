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
Provide various pytest utilities.
"""

import functools
import os
from typing import Callable

import pytest

PYTEST_LOG_SPEC_VAR = "bmq_log_levels"

skip_if_bmq_run_integration_test_not_set = pytest.mark.skipif(
    "BMQ_RUN_INTEGRATION_TESTS" not in os.environ,
    reason="BMQ_RUN_INTEGRATION_TESTS not set",
)

# Array of marks for files containing only integration tests.
integration_test_marks = [
    skip_if_bmq_run_integration_test_not_set,
    pytest.mark.integration_test,
]


def integration_test(test: Callable) -> Callable:
    """
    Mark 'test' to be skipped if 'BMQ_RUN_INTEGRATION_TESTS' is not set.
    """

    @skip_if_bmq_run_integration_test_not_set
    @pytest.mark.integration_test
    @functools.wraps(test)
    def conditional_test(*args, **kwargs):
        return test(*args, **kwargs)

    return conditional_test
