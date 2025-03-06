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

"""
Script for checking license header in C++/Python sources.
Usage:
python3 check_license.py [--path PATH]
optional arguments:
  --path PATH to the directory containing sources to check
"""
import argparse
import glob
from pathlib import Path
import re
from typing import List


def load_license_template(path: str) -> List[re.Pattern]:
    with open(path, "r") as f:
        lines = f.readlines()

    return [re.compile(line) for line in lines]


def check_license(fpath: str, expressions: List[re.Pattern]) -> bool:
    with open(fpath, "r") as f:
        ln = f.readline()
        if ln.startswith("#!"):  # skip possible shebang at the beginning of the file
            ln = f.readline()

        for expr in expressions:
            if not expr.match(ln):
                print(f"{fpath}:")
                print(ln)
                print("")
                return False
            ln = f.readline()
    return True


def main() -> None:
    parser = argparse.ArgumentParser(
        prog=f"python3 -m {check_license.__name__}",
        description="License checker",
    )

    parser.add_argument(
        "--path",
        default=".",
        type=str,
        action="store",
        metavar="PATH",
        help="PATH to the directory containing sources to check",
    )

    args = parser.parse_args()

    checks = {
        "*.cpp": "cxx_license_template.txt",
        "*.h": "cxx_license_template.txt",
        "*.c": "cxx_license_template.txt",
        "*.hpp": "cxx_license_template.txt",
        "*.py": "python_license_template.txt",
    }

    script_dir = Path(__file__).absolute().parent

    for ext, template in checks.items():
        fpaths = glob.glob(args.path + "/**/" + ext, recursive=True)
        fpaths.sort()
        if len(fpaths) == 0:
            continue

        expressions = load_license_template(script_dir.joinpath(template))
        for fpath in fpaths:
            check_license(fpath, expressions)


if __name__ == "__main__":
    main()
