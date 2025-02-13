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

import pytest
from typing import NamedTuple

# NOTE: don't use DOMAIN_* and URI_* constants (except DOMAIN_BROADCAST/URI_BROADCAST)
#  in the tests directly, or use them only for specific tests if needed.
# They are used for tests parametrization and passed to the tests via `domain_urls`
#  argument (see conftest.py).

DOMAIN_PRIORITY = "bmq.test.mmap.priority"
DOMAIN_PRIORITY_SC = "bmq.test.mmap.priority.sc"
DOMAIN_FANOUT = "bmq.test.mmap.fanout"
DOMAIN_FANOUT_SC = "bmq.test.mmap.fanout.sc"
DOMAIN_BROADCAST = "bmq.test.mem.broadcast"

TEST_QUEUE = "qqq"
TEST_APPIDS = ["foo", "bar", "baz"]

URI_PRIORITY = f"bmq://{DOMAIN_PRIORITY}/{TEST_QUEUE}"
URI_FANOUT = f"bmq://{DOMAIN_FANOUT}/{TEST_QUEUE}"
URI_FANOUT_FOO = f"bmq://{DOMAIN_FANOUT}/{TEST_QUEUE}?id=foo"
URI_FANOUT_BAR = f"bmq://{DOMAIN_FANOUT}/{TEST_QUEUE}?id=bar"
URI_FANOUT_BAZ = f"bmq://{DOMAIN_FANOUT}/{TEST_QUEUE}?id=baz"
URI_BROADCAST = f"bmq://{DOMAIN_BROADCAST}/{TEST_QUEUE}"

URI_PRIORITY_SC = f"bmq://{DOMAIN_PRIORITY_SC}/{TEST_QUEUE}"

URI_FANOUT_SC = f"bmq://{DOMAIN_FANOUT_SC}/{TEST_QUEUE}"
URI_FANOUT_SC_FOO = f"bmq://{DOMAIN_FANOUT_SC}/{TEST_QUEUE}?id=foo"
URI_FANOUT_SC_BAR = f"bmq://{DOMAIN_FANOUT_SC}/{TEST_QUEUE}?id=bar"
URI_FANOUT_SC_BAZ = f"bmq://{DOMAIN_FANOUT_SC}/{TEST_QUEUE}?id=baz"


class DomainUrls(NamedTuple):
    domain_priority: str
    domain_fanout: str
    uri_priority: str
    uri_fanout: str
    uri_fanout_foo: str
    uri_fanout_bar: str
    uri_fanout_baz: str


def eventual_consistency_param():
    return pytest.param(
        DomainUrls(
            domain_priority=DOMAIN_PRIORITY,
            domain_fanout=DOMAIN_FANOUT,
            uri_priority=URI_PRIORITY,
            uri_fanout=URI_FANOUT,
            uri_fanout_foo=URI_FANOUT_FOO,
            uri_fanout_bar=URI_FANOUT_BAR,
            uri_fanout_baz=URI_FANOUT_BAZ,
        ),
        id="eventual_consistency",
    )


def strong_consistency_param():
    return pytest.param(
        DomainUrls(
            domain_priority=DOMAIN_PRIORITY_SC,
            domain_fanout=DOMAIN_FANOUT_SC,
            uri_priority=URI_PRIORITY_SC,
            uri_fanout=URI_FANOUT_SC,
            uri_fanout_foo=URI_FANOUT_SC_FOO,
            uri_fanout_bar=URI_FANOUT_SC_BAR,
            uri_fanout_baz=URI_FANOUT_SC_BAZ,
        ),
        id="strong_consistency",
    )
