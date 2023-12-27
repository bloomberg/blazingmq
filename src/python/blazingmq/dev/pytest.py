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


DEFAULT_BATCH_SIZE = min(16, int(os.environ.get("PYTEST_XDIST_WORKER_COUNT", "1")))


def make_batch(num: int, maxbatch: int = DEFAULT_BATCH_SIZE) -> Callable[[], bool]:
    """
    Return a boolean function that returns True every 'num' times.
    """

    i = -1

    def batch():
        nonlocal i
        i += 1
        return i % maxbatch == num

    batch.__name__ = str(num)
    # to make pytest generate a parameter name in the form test_xxx[{num}]

    return batch


def batches(maxbatch: int = DEFAULT_BATCH_SIZE) -> List[Callable[[], bool]]:
    """
    Return a list of predicates useful for splitting long tests.

    Usage:

    @pytest.mark.parametrize("my_batch", batches())
    def test_big_data_set():
        for data in big_data_set:
            if not my_batch():
                continue
            ...
    """

    return [(make_batch(num, maxbatch)) for num in range(maxbatch)]


@pytest.fixture(params=batches())
def my_batch(request):
    """
    Fixture for splitting long tests.

    Usage:

    def test_big_data_set(my_batch):
        for data in big_data_set:
            if not my_batch():
                continue
            ...
    """

    return request.param
