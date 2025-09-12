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

from blazingmq.dev.it.process.client import _bool_lower, _build_command


def test_bool_lower():
    assert _bool_lower(True) == "true"
    assert _bool_lower(False) == "false"


def test_build_command():
    assert (
        _build_command("open", {"async": _bool_lower}, {"async_": True})
        == "open async=true"
    )
    assert (
        _build_command(
            "configure",
            {"maxUnconfirmedMessages": None},
            {"max_unconfirmed_messages": 42},
        )
        == "configure maxUnconfirmedMessages=42"
    )
    try:
        _build_command("configure", {}, {"foo": 42})
        assert False
    except Exception:  # pylint: disable=broad-exception-caught
        pass
