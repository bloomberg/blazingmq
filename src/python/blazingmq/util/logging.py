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

# pylint: disable=too-few-public-methods

import argparse
import logging
from pathlib import Path
import re


def level_value(level, error):
    if not hasattr(logging, level):
        raise ValueError(error.format(level=level))

    return getattr(logging, level)


def split_category_level(category_level):
    match = re.fullmatch(r"(\w+(?:\.\w+)*):(\w+)", category_level)

    if not match:
        raise ValueError(f'invalid category:level "{category_level}"')

    return match[1].lower(), match[2].upper()


def normalize_log_levels(level_spec):
    top_bmq_category_level = logging.getLogger().getEffectiveLevel()

    if not level_spec:
        return [top_bmq_category_level]

    category_levels = level_spec.split(",")

    if ":" not in category_levels[0]:
        top_bmq_category_level = level_value(
            category_levels.pop(0).upper(), 'invalid default level "{level}"'
        )

    return [top_bmq_category_level] + [
        (category, level_value(level, 'invalid level "{level}"'))
        for category, level in [
            split_category_level(category_level) for category_level in category_levels
        ]
    ]


def apply_normalized_log_levels(levels):
    """Parse 'level_spec' (a string formatted as defined at the top of the
    file) and set log levels accordingly.
    """

    top_level, *category_levels = levels
    logging.getLogger().setLevel(top_level)
    logging.getLogger("blazingmq").setLevel(top_level)

    for category_level in category_levels:
        category, level = category_level
        logging.getLogger(category).setLevel(level)


def apply_log_levels(level_spec):
    """Parse 'level_spec' (a string formatted as defined at the top of the
    file) and set log levels accordingly.
    """

    apply_normalized_log_levels(normalize_log_levels(level_spec))


class Action(argparse.Action):
    """An Action that sets the log levels according to the command-line switch
    value.
    """

    def __call__(self, parser, namespace, level_spec, option_string=None):
        levels = normalize_log_levels(level_spec)
        logging.basicConfig(level=levels[0])
        apply_normalized_log_levels(levels)


def make_parser(switches=None):
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

    def __init__(self, data, **kwargs):
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

    def __str__(self):
        import json

        return json.dumps(self.data, **{"indent": 2, **self.kwargs})


class lazy_pprint(LazyStr):
    """
    Lazily render data using standard 'pprint' module.
    """

    def __str__(self):
        import pprint

        return pprint.pformat(self.data, **{"indent": 2, "width": 80, **self.kwargs})


def _jsonable(data, depth, max_depth):
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

    def __str__(self):
        import json

        return json.dumps(
            _jsonable(self.data, 0, self.kwargs.pop("depth", None)),
            **{"indent": 2, **self.kwargs},
        )
