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
Provide a mechanism for setting log levels from the command line.

The format of the log level specification is: LEVEL[,CATEGORY:LEVEL]*. The
initial LEVEL is used to set the logging threshold . The subsequent
CATEGORY:LEVEL pairs are used to set the corresponding category-specific logger
levels.

Log levels can be specified in either upper or lower case. Category names are
converted to lower case.

Synopsis:

    # example.py

    parser = argparse.ArgumentParser(
        parents=[blazingmq.util.logging.make_parser(), ...],
        ...)

    $ ./example.py --log-level info
    $ ./example.py --log-level info,foo.bar:debug,bar.baz:trace
    $ ./example.py -l info,foo.bar:debug,bar.baz:trace

The module contains the following public function:

    - apply_log_levels -- Apply log levels according to specification.

    - make_parser -- Return a Parser object, capable of processing log related
        options.

The module contains the following public class:

    - Action -- A subclass of argparse.Action that parses the specified log
        levels and sets the logger levels accordingly.

"""

from __future__ import annotations

# pylint: disable=too-few-public-methods

import argparse
import logging
from pathlib import Path
import re
from typing import Any, Optional, Sequence, Union, cast


def level_value(level: str, error: str) -> int:
    if not hasattr(logging, level):
        raise ValueError(error.format(level=level))

    # Logging level constants are integers
    return cast(int, getattr(logging, level))


def split_category_level(category_level: str) -> tuple[str, str]:
    match = re.fullmatch(r"(\w+(?:\.\w+)*):(\w+)", category_level)

    if not match:
        raise ValueError(f'invalid category:level "{category_level}"')

    return match[1].lower(), match[2].upper()


def normalize_log_levels(
    level_spec: Optional[str],
) -> tuple[int, list[tuple[str, int]]]:
    top_bmq_category_level = logging.getLogger().getEffectiveLevel()

    if not level_spec:
        return (top_bmq_category_level, [])

    category_levels = level_spec.split(",")

    if ":" not in category_levels[0]:
        top_bmq_category_level = level_value(
            category_levels.pop(0).upper(), 'invalid default level "{level}"'
        )

    return (
        top_bmq_category_level,
        [
            (category, level_value(level, 'invalid level "{level}"'))
            for category, level in [
                split_category_level(category_level)
                for category_level in category_levels
            ]
        ],
    )


def apply_normalized_log_levels(levels: tuple[int, list[tuple[str, int]]]) -> None:
    """Parse 'level_spec' (a string formatted as defined at the top of the
    file) and set log levels accordingly.
    """

    top_level, category_levels = levels
    logging.getLogger().setLevel(top_level)
    logging.getLogger("blazingmq").setLevel(top_level)

    for category_level in category_levels:
        category, level = category_level
        logging.getLogger(category).setLevel(level)


def apply_log_levels(level_spec: Optional[str]) -> None:
    """Parse 'level_spec' (a string formatted as defined at the top of the
    file) and set log levels accordingly.
    """

    apply_normalized_log_levels(normalize_log_levels(level_spec))


class Action(argparse.Action):
    """An Action that sets the log levels according to the command-line switch
    value.
    """

    def __init__(
        self,
        option_strings: Sequence[str],
        dest: str,
        nargs: Optional[Union[int, str]] = None,
        **kwargs: Any,
    ) -> None:
        # Only one arg value allowed, to safely cast to str
        if nargs is not None:
            raise ValueError("nargs not allowed")
        super().__init__(option_strings, dest, **kwargs)

    def __call__(
        self,
        parser: argparse.ArgumentParser,
        namespace: argparse.Namespace,
        values: Union[str, Sequence[Any], None],
        option_string: Optional[str] = None,
    ) -> None:
        # values is always str because only one value allowed in __init__
        assert isinstance(values, str)
        setattr(namespace, self.dest, values)
        levels = normalize_log_levels(values)
        logging.basicConfig(level=levels[0])
        apply_normalized_log_levels(levels)


def make_parser(switches: Optional[list[str]] = None) -> argparse.ArgumentParser:
    """Return a Parser object, intended to be used as a parent parser.

    Keyword Arguments:
        - switches -- A list of strings, to be added to the command line
            interface. If not specified, defaults to '--log-level' and '-l'.
    """

    parser = argparse.ArgumentParser(add_help=False)

    parser.add_argument(
        *(["--log-level", "-l"] if switches is None else switches),
        action=Action,
        metavar="LEVELS",
        help="set log level(s) as specified by LEVELS",
    )

    return parser


class LazyStr:
    """Store the arguments for a function that formats nested data structures
    to a string.

    Instances of LazyStr subclasses are intended to wrap a data structure, and
    some formatting options, and render the data as a string only if it is
    needed.  Typically LazyStr objects will be used as arguments to logging
    functions, which expect a format and a list of arguments.  The arguments
    are always evaluated, but converted to a string only if the format is
    actually converted to a string (depending on the logger's level).
    Rendering complex data via 'json' or 'prettyprinter' is costly, so it
    should be done only as necessary.  LazyStr subclasses simply wrap their
    arguments, and defer rendering the data until it is actually needed.
    """

    def __init__(self, data: Any, **kwargs: Any) -> None:
        self.data = data
        self.kwargs = kwargs


# Names of following classes diverge from PEP8 because they are meant to be
# used like functions.  It would be trivial to create small functions that
# create an instance of the class.  It would also be pointlessly dogmatic.
# pylint: disable=invalid-name

# Since the following code implements lazy behavior, let us load the
# dependencies lazily.
# pylint: disable=import-outside-toplevel


class lazy_json(LazyStr):
    """
    Lazily render data to JSON.
    """

    def __str__(self) -> str:
        import json

        return json.dumps(self.data, **{"indent": 2, **self.kwargs})


class lazy_pprint(LazyStr):
    """
    Lazily render data using standard 'pprint' module.
    """

    def __str__(self) -> str:
        import pprint

        return pprint.pformat(self.data, **{"indent": 2, "width": 80, **self.kwargs})


def _jsonable(data: Any, depth: int, max_depth: Optional[int]) -> Any:
    if isinstance(data, str):
        return data

    if max_depth is not None and depth == max_depth:
        return str(data)

    if isinstance(data, dict):
        return {str(k): _jsonable(v, depth + 1, max_depth) for k, v in data.items()}

    if hasattr(data, "_asdict"):
        return {
            str(k): _jsonable(v, depth + 1, max_depth)
            for k, v in data._asdict().items()
        }

    if hasattr(data, "__iter__"):
        return [_jsonable(v, depth + 1, max_depth) for v in iter(data)]

    if isinstance(data, Path):
        return str(data)

    return str(data)


class lazy_data(LazyStr):
    """
    Recursively render 'data' to JSON.  If 'data' contains values that
    have a '_hasdict' method (typically, NamedTuples), they are rendered as
    dictionaries.  Recursion is limited to 'depth', if specified.  Other
    keyword arguments are passed to 'json.dumps'.

    Notes:
    - The rendering is lazy.
    - Recursive data structures are not supported.
    """

    def __str__(self) -> str:
        import json

        return json.dumps(
            _jsonable(self.data, 0, self.kwargs.pop("depth", None)),
            **{"indent": 2, **self.kwargs},
        )
