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

"""
Script for printing core dump contents.
Usage:
python3 print_cores.py [--cores-dir CORES_PATH] [--bin-path BIN_PATH]
optional arguments:
  --cores-dir CORES_DIR path to the directory containing core dumps
  --bin-path BIN_PATH path to the executable
"""

import argparse
import glob
from pathlib import Path
import re
import subprocess


def is_core(path: str) -> bool:
    return re.match(r".+\.\d+\.\d+\.\d+", str(path)) is not None


def main() -> None:
    parser = argparse.ArgumentParser(
        prog="python3 print_cores.py",
        description="Core dumps printer",
    )

    parser.add_argument(
        "--cores-dir",
        default=".",
        type=str,
        action="store",
        metavar="CORES_DIR",
        help="CORES_DIR path to the directory containing core dumps",
    )

    parser.add_argument(
        "--bin-path",
        default=".",
        type=str,
        action="store",
        metavar="BIN_PATH",
        help="BIN_PATH path to the executable",
    )

    args = parser.parse_args()

    fpaths = glob.glob(str(Path(args.cores_dir).joinpath("*")), recursive=True)
    fpaths = [path for path in fpaths if is_core(path)]
    fpaths.sort()

    if len(fpaths) == 0:
        print(f"No cores found in directory: {args.cores_dir}")

    for core_path in fpaths:
        cmd = [
            "gdb",
            "-q",
            "-batch",
            "-ex",
            "bt full",
            args.bin_path,
            core_path,
        ]

        print(f"::group::{core_path}", flush=True)
        print(f"#CORE: {core_path}", flush=True)
        print("#COMMAND: " + " ".join(cmd), flush=True)

        subprocess.run(cmd, check=True)

        print("::endgroup::", flush=True)


if __name__ == "__main__":
    main()
