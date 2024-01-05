"""
Provide various pytest utilities.
"""

import functools
import os
from typing import Callable, List

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
