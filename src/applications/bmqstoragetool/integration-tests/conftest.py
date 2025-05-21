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

from pathlib import Path
import pytest
from blazingmq.dev import paths


@pytest.fixture
def storagetool() -> Path:
    return paths.required_paths.storagetool


@pytest.fixture
def journal_file() -> Path:
    return (
        paths.required_paths.repository
        / "src/applications/bmqstoragetool/integration-tests/fixtures/test.bmq_journal"
    )


@pytest.fixture
def journal_path() -> Path:
    return (
        paths.required_paths.repository
        / "src/applications/bmqstoragetool/integration-tests/fixtures/test.*"
    )


@pytest.fixture
def data_file() -> Path:
    return (
        paths.required_paths.repository
        / "src/applications/bmqstoragetool/integration-tests/fixtures/test.bmq_data"
    )


@pytest.fixture
def csl_file() -> Path:
    return (
        paths.required_paths.repository
        / "src/applications/bmqstoragetool/integration-tests/fixtures/test.bmq_csl"
    )


def _load_expected_result_file(file_path) -> bytes:
    with open(file_path, "r", encoding="utf8") as file:
        return file.read().encode()


@pytest.fixture
def expected_short_result() -> bytes:
    short_res_file = (
        paths.required_paths.repository
        / "src/applications/bmqstoragetool/integration-tests/fixtures/short_result.txt"
    )
    return _load_expected_result_file(short_res_file)


@pytest.fixture
def expected_csl_short_result() -> bytes:
    short_res_file = (
        paths.required_paths.repository
        / "src/applications/bmqstoragetool/integration-tests/fixtures/short_csl_result.txt"
    )
    return _load_expected_result_file(short_res_file)


@pytest.fixture
def expected_detail_result() -> bytes:
    details_res_file = (
        paths.required_paths.repository
        / "src/applications/bmqstoragetool/integration-tests/fixtures/detail_result.txt"
    )
    return _load_expected_result_file(details_res_file)


@pytest.fixture
def expected_csl_detail_result() -> bytes:
    details_res_file = (
        paths.required_paths.repository
        / "src/applications/bmqstoragetool/integration-tests/fixtures/detail_csl_result.txt"
    )
    return _load_expected_result_file(details_res_file)


@pytest.fixture
def expected_payload_dump() -> bytes:
    payload_dump_file = (
        paths.required_paths.repository
        / "src/applications/bmqstoragetool/integration-tests/fixtures/payload_dump.txt"
    )
    return _load_expected_result_file(payload_dump_file)


@pytest.fixture
def expected_summary_result() -> bytes:
    summary_result_file = (
        paths.required_paths.repository
        / "src/applications/bmqstoragetool/integration-tests/fixtures/summary_result.txt"
    )
    return _load_expected_result_file(summary_result_file)


@pytest.fixture
def expected_csl_summary_result() -> bytes:
    summary_result_file = (
        paths.required_paths.repository
        / "src/applications/bmqstoragetool/integration-tests/fixtures/summary_csl_result.txt"
    )
    return _load_expected_result_file(summary_result_file)


@pytest.fixture
def expected_summary_result_with_queue_info() -> bytes:
    summary_result_with_queue_info_file = (
        paths.required_paths.repository
        / "src/applications/bmqstoragetool/integration-tests/fixtures/summary_result_with_queue_info.txt"
    )
    return _load_expected_result_file(summary_result_with_queue_info_file)


@pytest.fixture
def expected_queueop_result() -> bytes:
    queueop_result_file = (
        paths.required_paths.repository
        / "src/applications/bmqstoragetool/integration-tests/fixtures/queueop_result.txt"
    )
    return _load_expected_result_file(queueop_result_file)


@pytest.fixture
def expected_journalop_result() -> bytes:
    journalop_result_file = (
        paths.required_paths.repository
        / "src/applications/bmqstoragetool/integration-tests/fixtures/journalop_result.txt"
    )
    return _load_expected_result_file(journalop_result_file)


@pytest.fixture
def expected_queueop_journalop_summary_result() -> bytes:
    summary_queueop_journalop_result = (
        paths.required_paths.repository
        / "src/applications/bmqstoragetool/integration-tests/fixtures/summary_queueop_journalop_result.txt"
    )
    return _load_expected_result_file(summary_queueop_journalop_result)
