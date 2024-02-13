"""Test suite for blazingmq.util.logging.
"""

from logging import DEBUG, INFO, getLogger
from unittest.mock import patch

import blazingmq.util.logging
import pytest

DEFAULT_LEVEL = getLogger().getEffectiveLevel()


@pytest.mark.parametrize(
    "spec,expected",
    [
        (None, [DEFAULT_LEVEL]),
        ("INFO", [INFO]),
        ("DEBUG", [DEBUG]),
        ("blazingmq.foo:DEBUG", [DEFAULT_LEVEL, ("blazingmq.foo", DEBUG)]),
        (
            "blazingmq.foo:DEBUG,blazingmq.bar:INFO",
            [DEFAULT_LEVEL, ("blazingmq.foo", DEBUG), ("blazingmq.bar", INFO)],
        ),
        ("DEBUG,blazingmq.foo:INFO", [DEBUG, ("blazingmq.foo", INFO)]),
    ],
)
def test_normalize_levels(spec, expected):
    assert blazingmq.util.logging.normalize_log_levels(spec) == expected


def test_logging_parser():
    """Tests"""

    parser = blazingmq.util.logging.make_parser()

    with patch.object(getLogger("blazingmq"), "setLevel") as logger_mock:
        parser.parse_args(["--log-level", "info"])
        logger_mock.assert_called_with(INFO)

    with patch.object(getLogger("blazingmq"), "setLevel") as top_logger_mock, patch.object(
        getLogger("foo.bar"), "setLevel"
    ) as logger_mock:
        parser.parse_args(["--log-level", "info,foo.bar:debug"])
        top_logger_mock.assert_called_with(INFO)
        logger_mock.assert_called_with(DEBUG)

    with pytest.raises(ValueError, match="invalid category:level"):
        parser.parse_args(["-l", "info,foo.bar"])

    with pytest.raises(ValueError, match="invalid default level"):
        parser.parse_args(["-l", "not_a_level"])
