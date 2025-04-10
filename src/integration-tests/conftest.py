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

import contextlib
import logging

import pytest

import blazingmq.dev.it.logging
import blazingmq.dev.it.testconstants as tc
import blazingmq.util.logging as bul
from blazingmq.dev.pytest import PYTEST_LOG_SPEC_VAR


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

    help_ = "run only with the specified order"
    parser.addoption(
        "--bmq-wave",
        type=int,
        action="store",
        metavar="WAVE",
        help=help_,
    )


def pytest_configure(config):
    logging.setLoggerClass(blazingmq.dev.it.logging.BMQLogger)

    level_spec = config.getoption(PYTEST_LOG_SPEC_VAR) or config.getini(
        PYTEST_LOG_SPEC_VAR
    )

    if level_spec:
        levels = bul.normalize_log_levels(level_spec)
        bul.apply_normalized_log_levels(levels)
        top_level = levels[0]
        logging.getLogger("proc").setLevel(top_level)
        logging.getLogger("test").setLevel(top_level)


def pytest_collection_modifyitems(config, items):
    active_wave = config.getoption("bmq_wave")
    if active_wave is None:
        return

    for item in items:
        mark = None
        for mark in item.iter_markers(name="order"):
            pass

        if mark is None:
            order = 0
        else:
            order = int(mark.args[0])

        if order == active_wave:
            continue

        item.add_marker(
            pytest.mark.skip(reason=f"order = {order}, running {active_wave} only")
        )


@pytest.fixture
def ec_domain_urls():
    return tc.EC_DOMAIN_URLS


@pytest.fixture
def sc_domain_urls():
    return tc.SC_DOMAIN_URLS


@pytest.fixture(
    params=[
        pytest.param(
            tc.SC_DOMAIN_URLS,
            id="strong_consistency",
            marks=[pytest.mark.strong_consistency],
        ),
        pytest.param(
            tc.EC_DOMAIN_URLS,
            id="eventual_consistency",
            marks=[pytest.mark.eventual_consistency],
        ),
    ]
)
def domain_urls(request):
    return request.param
