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


def test_short_result(storagetool, csl_file, expected_csl_short_result):
    """
    This test:
     - checks that storage tool can process CSL file and output short result.
     - checks that storage tool can process CSL file from the beginning.
    """
    res = subprocess.run(
        [storagetool, "--csl-file", csl_file], capture_output=True, check=True
    )
    assert res.returncode == 0
    assert res.stdout == expected_csl_short_result

    res = subprocess.run(
        [storagetool, "--csl-file", csl_file, "--csl-from-begin"],
        capture_output=True,
        check=True,
    )
    assert res.returncode == 0
    assert re.search(b"2 snapshot record", res.stdout) is not None


def test_short_json(storagetool, csl_file):
    """
    This test:
     - checks that storage tool can process CSL file and output short result in JSON (pretty and line) format.
    """
    for mode in ["pretty", "line"]:
        res = subprocess.run(
            [storagetool, "--csl-file", csl_file, f"--print-mode=json-{mode}"],
            capture_output=True,
            check=True,
        )
        assert res.returncode == 0
        json_res = json.loads(res.stdout)
        assert json_res["SnapshotRecords"] == "1"
        assert json_res["CommitRecords"] == "1"
        assert len(json_res["Records"]) == 2


def test_detail_result(storagetool, csl_file, expected_csl_detail_result):
    """
    This test:
     - checks that storage tool can process CSL file and output detail result.
    """
    res = subprocess.run(
        [storagetool, "--csl-file", csl_file, "--details"],
        capture_output=True,
        check=True,
    )
    assert res.returncode == 0
    assert res.stdout == expected_csl_detail_result


def test_detail_json(storagetool, csl_file):
    """
    This test:
     - checks that storage tool can process CSL file and output detail result in JSON (pretty and line) format.
    """
    for mode in ["pretty", "line"]:
        res = subprocess.run(
            [
                storagetool,
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
        assert json_res["SnapshotRecords"] == "1"
        assert json_res["CommitRecords"] == "1"
        assert len(json_res["Records"]) == 2


def test_summary_result(storagetool, csl_file, expected_csl_summary_result):
    """
    This test:
     - checks that storage tool can process CSL file and output summary result.
    """
    res = subprocess.run(
        [storagetool, "--csl-file", csl_file, "--summary"],
        capture_output=True,
        check=True,
    )
    assert res.returncode == 0
    assert res.stdout == expected_csl_summary_result


def test_summary_json(storagetool, csl_file):
    """
    This test:
     - checks that storage tool can process CSL file and output summary result.
    """
    for mode in ["pretty", "line"]:
        res = subprocess.run(
            [
                storagetool,
                "--csl-file",
                csl_file,
                "--summary",
                f"--print-mode=json-{mode}",
            ],
            capture_output=True,
            check=True,
        )
        assert res.returncode == 0
        json_res = json.loads(res.stdout)
        assert "Summary" in json_res
        assert json_res["Summary"]["SnapshotRecords"] == "1"
        assert json_res["Summary"]["CommitRecords"] == "1"
        assert "Queues" in json_res
        assert len(json_res["Queues"]) == 1


def test_search_range(storagetool, csl_file):
    """
    This test:
     - checks that storage tool can process CSL file and search records
     by time, offset and sequence number ranges.
    """

    # Search by time range
    res = subprocess.run(
        [
            storagetool,
            "--csl-file",
            csl_file,
            "--csl-from-begin",
            "--timestamp-gt=1730210423",
            "--timestamp-lt=1730210574",
        ],
        capture_output=True,
        check=True,
    )
    assert res.returncode == 0
    assert re.search(b"1 update record", res.stdout) is not None
    assert re.search(b"1 commit record", res.stdout) is not None
    assert re.search(b"No snapshot record", res.stdout) is not None

    # Search by offset range
    res = subprocess.run(
        [
            storagetool,
            "--csl-file",
            csl_file,
            "--csl-from-begin",
            "--offset-gt=88",
            "--offset-lt=388",
        ],
        capture_output=True,
        check=True,
    )
    assert res.returncode == 0
    assert re.search(b"1 update record", res.stdout) is not None
    assert re.search(b"1 commit record", res.stdout) is not None
    assert re.search(b"No snapshot record", res.stdout) is not None

    # Search by sequence number range
    res = subprocess.run(
        [
            storagetool,
            "--csl-file",
            csl_file,
            "--csl-from-begin",
            "--offset-gt=88",
            "--offset-lt=388",
        ],
        capture_output=True,
        check=True,
    )
    assert res.returncode == 0
    assert re.search(b"1 update record", res.stdout) is not None
    assert re.search(b"1 commit record", res.stdout) is not None
    assert re.search(b"No snapshot record", res.stdout) is not None


def test_search_offset(storagetool, csl_file):
    """
    This test:
     - checks that storage tool can process CSL file and search records by offset.
     - checks that warning is printed if non existing offset is passed.
    """
    res = subprocess.run(
        [storagetool, "--csl-file", csl_file, "--csl-from-begin", "--offset=316"],
        capture_output=True,
        check=True,
    )
    assert res.returncode == 0
    assert re.search(b"1 commit record", res.stdout) is not None
    assert re.search(b"No update record", res.stdout) is not None
    assert re.search(b"No snapshot record", res.stdout) is not None

    res = subprocess.run(
        [storagetool, "--csl-file", csl_file, "--csl-from-begin", "--offset=317"],
        capture_output=True,
        check=True,
    )
    assert res.returncode == 0
    assert re.search(b"No update record", res.stdout) is not None
    assert re.search(b"No commit record", res.stdout) is not None
    assert re.search(b"No snapshot record", res.stdout) is not None
    assert re.search(b"The following 1 offset", res.stdout) is not None


def test_search_seqnum(storagetool, csl_file):
    """
    This test:
     - checks that storage tool can process CSL file and search records by sequence number.
     - checks that warning is printed if non existing sequence number is passed.
    """
    res = subprocess.run(
        [storagetool, "--csl-file", csl_file, "--csl-from-begin", "--seqnum=1-4"],
        capture_output=True,
        check=True,
    )
    assert res.returncode == 0
    assert re.search(b"1 commit record", res.stdout) is not None
    assert re.search(b"No update record", res.stdout) is not None
    assert re.search(b"No snapshot record", res.stdout) is not None

    res = subprocess.run(
        [storagetool, "--csl-file", csl_file, "--csl-from-begin", "--seqnum=1-7"],
        capture_output=True,
        check=True,
    )
    assert res.returncode == 0
    assert re.search(b"No update record", res.stdout) is not None
    assert re.search(b"No commit record", res.stdout) is not None
    assert re.search(b"No snapshot record", res.stdout) is not None
    assert re.search(b"The following 1 sequence number", res.stdout) is not None
