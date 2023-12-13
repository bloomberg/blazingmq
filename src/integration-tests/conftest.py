import contextlib
import logging

import bmq.dev.it.logging
import bmq.util.logging as bul
from bmq.dev.pytest import PYTEST_LOG_SPEC_VAR


def pytest_addoption(parser):
    # Do this here so the 'TRACE' level is available in the command line
    # switches.
    logging.TRACE = logging.DEBUG - 1
    logging.addLevelName(logging.TRACE, "TRACE")

    help_ = "put combined broker logs and client outputs for failed tests in DIR"
    parser.addoption(
        "--bmq-log-dir", dest="bmq_log_dir", action="store", metavar="DIR", help=help_
    )
    parser.addini("bmq_log_dir", help_, type=None, default=None)

    help_ = "use POLICY in cluster configuration where available"
    parser.addoption(
        "--bmq-cluster-policy",
        dest="bmq_cluster_policy",
        action="store",
        metavar="POLICY",
        help=help_,
    )
    parser.addini("bmq_cluster_policy", help_, type=None, default=None)

    help_ = "tack ARGS at the end of the bmqbrkr command line"
    parser.addoption(
        "--bmq-broker-extra-args",
        dest="bmq_broker_extra_args",
        action="store",
        metavar="ARGS",
        help=help_,
    )
    parser.addini("bmq_broker_extra_args", help_, type=None, default=None)

    help_ = "always keep logs"
    parser.addoption(
        "--bmq-keep-logs",
        dest="bmq_keep_logs",
        action="store_true",
        default=False,
        help=help_,
    )
    parser.addini("bmq_keep_logs", help_, type=None, default=None)

    help_ = "do not delete work directory after test finishes"
    parser.addoption(
        "--bmq-keep-work-dirs",
        dest="bmq_keep_work_dirs",
        action="store_true",
        default=False,
        help=help_,
    )
    parser.addini("bmq_keep_work_dirs", help_, type=None, default=None)

    help_ = "break before test begins - NOTE: requires -s, no docker, no parallelism"
    parser.addoption(
        "--bmq-break-before-test",
        dest="bmq_break_before_test",
        action="store_true",
        default=False,
        help=help_,
    )

    help_ = "do not report process crashes or errors during shutdown"
    parser.addoption(
        "--bmq-tolerate-dirty-shutdown",
        dest="bmq_tolerate_dirty_shutdown",
        action="store_true",
        default=False,
        help=help_,
    )
    parser.addini("bmq_tolerate_dirty_shutdown", help_, type=None, default=False)

    with contextlib.suppress(Exception):
        help_ = "set per-category log level"
        parser.addoption(
            "--bmq-log-level",
            dest=PYTEST_LOG_SPEC_VAR,
            action="store",
            default=[],
            metavar="LEVEL",
            help=help_,
        )
        parser.addini(PYTEST_LOG_SPEC_VAR, help_, type=None, default=None)


def pytest_configure(config):
    logging.setLoggerClass(bmq.dev.it.logging.BMQLogger)

    level_spec = config.getoption(PYTEST_LOG_SPEC_VAR) or config.getini(
        PYTEST_LOG_SPEC_VAR
    )

    if level_spec:
        levels = bul.normalize_log_levels(level_spec)
        bul.apply_normalized_log_levels(levels)
        top_level = levels[0]
        logging.getLogger("proc").setLevel(top_level)
        logging.getLogger("test").setLevel(top_level)
