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

import subprocess
import re
import json


def test_short_result(storagetool, journal_file, expected_short_result):
    """
    This test:
     - checks that storage tool can process journal file and output messages short result (message GUIDs).
     - checks GUID searching.
    """
    res = subprocess.run(
        [storagetool, "--journal-file", journal_file], capture_output=True, check=True
    )
    assert res.returncode == 0
    assert res.stdout == expected_short_result

    res = subprocess.run(
        [
            storagetool,
            "--journal-file",
            journal_file,
            "--guid",
            "40000000000215B2967EEDFA1085BA02",
        ],
        capture_output=True,
        check=True,
    )
    assert res.returncode == 0
    assert re.search(b"40000000000215B2967EEDFA1085BA02", res.stdout) is not None
    assert re.search(b"400000000002B471F5B3AC11AA7D7DAB", res.stdout) is None


def test_short_json(storagetool, journal_file):
    """
    This test:
     - checks that storage tool can process journal file and output messages short result in JSON (pretty and line) format.
    """
    for mode in ["pretty", "line"]:
        res = subprocess.run(
            [storagetool, "--journal-file", journal_file, f"--print-mode=json-{mode}"],
            capture_output=True,
            check=True,
        )
        assert res.returncode == 0
        json_res = json.loads(res.stdout)
        assert json_res["TotalMessages"] == "2"
        assert "40000000000215B2967EEDFA1085BA02" in json_res["Records"]
        assert "400000000002B471F5B3AC11AA7D7DAB" in json_res["Records"]


def test_detail_result(storagetool, journal_file, csl_file, expected_detail_result):
    """
    This test:
     - checks that storage tool can process journal file and output messages detail result.
     - checks that storage tool can process journal and csl files and output details with queue names.
    """
    res = subprocess.run(
        [storagetool, "--journal-file", journal_file, "--details"],
        capture_output=True,
        check=True,
    )
    assert res.returncode == 0
    assert res.stdout == expected_detail_result

    res = subprocess.run(
        [
            storagetool,
            "--journal-file",
            journal_file,
            "--csl-file",
            csl_file,
            "--details",
        ],
        capture_output=True,
        check=True,
    )
    assert res.returncode == 0
    assert (
        re.search(
            r"QueueUri\s+: bmq://bmq.test.persistent.priority/my-first-queue",
            res.stdout.decode(),
        )
        is not None
    )


def test_detail_json(storagetool, journal_file, csl_file):
    """
    This test:
     - checks that storage tool can process journal file and output result in JSON (pretty and line) format.
    """
    for mode in ["pretty", "line"]:
        res = subprocess.run(
            [
                storagetool,
                "--journal-file",
                journal_file,
                "--csl-file",
                csl_file,
                "--details",
                f"--print-mode=json-{mode}",
            ],
            capture_output=True,
            check=True,
        )
        assert res.returncode == 0
        json_res = json.loads(res.stdout)
        assert json_res["TotalMessages"] == "2"
        assert len(json_res["Records"]) == 4


def test_payload_dump(
    storagetool, journal_path, journal_file, data_file, expected_payload_dump
):
    """
    This test:
     - checks that storage tool can process journal and data files and dump message payload.
     - checks that storage tool can apply dump limit.
    """
    res = subprocess.run(
        [
            storagetool,
            "--journal-file",
            journal_file,
            "--data-file",
            data_file,
            "--dump-payload",
        ],
        capture_output=True,
        check=True,
    )
    assert res.returncode == 0
    assert res.stdout == expected_payload_dump

    res = subprocess.run(
        [storagetool, "--journal-path", journal_path, "--dump-payload"],
        capture_output=True,
        check=True,
    )
    assert res.returncode == 0
    assert res.stdout == expected_payload_dump

    res = subprocess.run(
        [
            storagetool,
            "--journal-path",
            journal_path,
            "--dump-payload",
            "--dump-limit=5",
        ],
        capture_output=True,
        check=True,
    )
    assert res.returncode == 0
    assert re.search(b"First 5 bytes of payload:", res.stdout) is not None


def test_summary_result(storagetool, journal_path, csl_file, expected_summary_result):
    """
    This test checks that storage tool can process journal file and output messages summary result.
    """
    res = subprocess.run(
        [
            storagetool,
            "--journal-path",
            journal_path,
            "--csl-file",
            csl_file,
            "--summary",
        ],
        capture_output=True,
        check=True,
    )
    assert res.returncode == 0
    assert res.stdout == expected_summary_result


def test_summary_result_with_queue_info(
    storagetool, journal_path, csl_file, expected_summary_result_with_queue_info
):
    """
    This test checks that storage tool can process journal file and output messages summary result with queue info.
    """
    res = subprocess.run(
        [
            storagetool,
            "--journal-path",
            journal_path,
            "--csl-file",
            csl_file,
            "--summary",
            "--min-records-per-queue",
            "1",
        ],
        capture_output=True,
        check=True,
    )
    assert res.returncode == 0
    assert res.stdout == expected_summary_result_with_queue_info


def test_summary_with_queue_info_json(storagetool, journal_path, csl_file):
    """
    This test checks that storage tool can process journal file and output summary result in JSON (pretty and line) format.
    """
    for mode in ["pretty", "line"]:
        res = subprocess.run(
            [
                storagetool,
                "--journal-path",
                journal_path,
                "--csl-file",
                csl_file,
                "--summary",
                "--min-records-per-queue",
                "1",
                f"--print-mode=json-{mode}",
            ],
            capture_output=True,
            check=True,
        )

        assert res.returncode == 0

        json_res = json.loads(res.stdout)
        assert json_res["TotalMessagesNumber"] == "2"
        assert json_res["PartiallyConfirmedMessagesNumber"] == "0"
        assert json_res["ConfirmedMessagesNumber"] == "1"
        assert json_res["OutstandingMessagesNumber"] == "1"
        assert json_res["OutstandingRatio"] == "50"
        assert json_res["TotalRecordsNumber"] == "4"
        assert len(json_res["PerQueueRecordsNumber"]) == 1
        assert json_res["PerQueueRecordsNumber"][0]["Queue Key"] == "26DACDC974"
        assert "JournalFileDetails" in json_res
        assert "DataFileDetails" in json_res


def test_confirmed_outstanding_result(storagetool, journal_file):
    """
    This test:
     - checks that storage tool can search confirmed messages and output short result (message GUIDs).
     - checks that storage tool can search outstanding messages and output short result (message GUIDs).
    """
    res = subprocess.run(
        [storagetool, "--journal-file", journal_file, "--confirmed"],
        capture_output=True,
        check=True,
    )
    assert res.returncode == 0
    assert re.search(b"400000000002B471F5B3AC11AA7D7DAB", res.stdout) is not None
    assert re.search(b"40000000000215B2967EEDFA1085BA02", res.stdout) is None

    res = subprocess.run(
        [storagetool, "--journal-file", journal_file, "--outstanding"],
        capture_output=True,
        check=True,
    )
    assert res.returncode == 0
    assert re.search(b"40000000000215B2967EEDFA1085BA02", res.stdout) is not None
    assert re.search(b"400000000002B471F5B3AC11AA7D7DAB", res.stdout) is None


def test_queueop_result(storagetool, journal_file, expected_queueop_result):
    """
    This test checks that storage tool can process journal file and output queueOp records short result.
    """
    res = subprocess.run(
        [storagetool, "--journal-file", journal_file, "-r=queue-op"],
        capture_output=True,
        check=True,
    )
    assert res.returncode == 0
    assert res.stdout == expected_queueop_result


def test_journalop_result(storagetool, journal_file, expected_journalop_result):
    """
    This test checks that storage tool can process journal file and output journalOp records short result.
    """
    res = subprocess.run(
        [storagetool, "--journal-file", journal_file, "-r=journal-op"],
        capture_output=True,
        check=True,
    )
    assert res.returncode == 0
    assert res.stdout == expected_journalop_result


def test_queueop_journalop_json(storagetool, journal_file):
    """
    This test checks that storage tool can process journal file and output queueOp and journalOp records in JSON (pretty and line) format.
    """
    for mode in ["pretty", "line"]:
        res = subprocess.run(
            [
                storagetool,
                "--journal-file",
                journal_file,
                "-r=queue-op",
                "-r=journal-op",
                f"--print-mode=json-{mode}",
            ],
            capture_output=True,
            check=True,
        )
        assert res.returncode == 0
        json_res = json.loads(res.stdout)
        assert json_res["QueueOpRecords"] == "1"
        assert json_res["JournalOpRecords"] == "8"
        assert len(json_res["Records"]) == 9


def test_queueop_journalop_summary_result(
    storagetool, journal_file, expected_queueop_journalop_summary_result
):
    """
    This test checks that storage tool can process journal file and output queueOp and journalOp records summary result.
    """
    res = subprocess.run(
        [
            storagetool,
            "--journal-file",
            journal_file,
            "-r=queue-op",
            "-r=journal-op",
            "--summary",
        ],
        capture_output=True,
        check=True,
    )
    assert res.returncode == 0
    assert res.stdout == expected_queueop_journalop_summary_result


def test_queueop_journalop_summary_json(storagetool, journal_file):
    """
    This test checks that storage tool can process journal file and output queueOp and journalOp records summary result in JSON (pretty and line) format.
    """
    for mode in ["pretty", "line"]:
        res = subprocess.run(
            [
                storagetool,
                "--journal-file",
                journal_file,
                "-r=queue-op",
                "-r=journal-op",
                "--summary",
                f"--print-mode=json-{mode}",
            ],
            capture_output=True,
            check=True,
        )
        assert res.returncode == 0
        json_res = json.loads(res.stdout)
        assert json_res["TotalQueueOperationsNumber"] == "1"
        assert json_res["CreationOperationsNumber"] == "1"
        assert json_res["JournalOperationsNumber"] == "8"
        assert json_res["TotalRecordsNumber"] == "9"
        assert "JournalFileDetails" in json_res
        assert "DataFileDetails" not in json_res
