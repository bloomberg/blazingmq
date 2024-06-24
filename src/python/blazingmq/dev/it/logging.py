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

# This contains the bmqit configuration; set in 'conftest.py' and used in
# 'fixtures'py.

import inspect
import logging


def clip(text, width):
    """
    Return 'text' as a string of length 'width'. If length of 'text' exceeds
    'width', replace the beginning of 'text' with a '*' to make it fit.
    """
    n = len(text)
    return (
        "*" + text[n - width + 1 :] if len(text) > width else text + " " * (width - n)
    )


class BMQLogger(logging.getLoggerClass()):
    def makeRecord(self, name, level, fn, lno, msg, args, exc_info, func, extra, sinfo):
        # Add two attributes.
        extra = {
            "bmqprocess": "N/A",  # bmqbrkr or bmqtool process name, test id
            **(extra or {}),
        }
        overrides = extra.pop("ball_overrides", {})
        # For bmqit-generated attributes, plus standard name, filename and
        # message attributes, add several width-limited versions,
        # e.g. 'name12', which contains the category name in 12 characters,
        # clipped if necessary.
        extra = {
            **extra,
            **{
                f"{key}{width}": clip(value, width)
                for width in (8, 16, 24, 32)
                for key, value in {
                    **extra,
                    "name": name,
                    "filename": fn,
                    "msg": msg,
                }.items()
            },
        }
        record = super().makeRecord(
            name, level, fn, lno, msg, args, exc_info, func, extra, sinfo
        )
        # Override standard record attributes with corresponding values from
        # BALL.
        for attr, value in overrides.items():
            assert hasattr(record, attr)
            if value is not None:
                setattr(record, attr, value)
        return record


class BallLoggerAdapter(logging.LoggerAdapter):
    def process(self, msg, kwargs):
        try:
            location = None
            target = self.extra.get("blp_log_from")
            frames = inspect.stack()

            for i, frame in enumerate(frames):
                if frame.filename == target:
                    location = frames[i + 1]

            if location is not None:
                ball_overrides = self.extra.setdefault("ball_overrides", {})
                ball_overrides["filename"] = location.filename.split("/")[-1]
                ball_overrides["lineno"] = location.lineno

            return super().process(msg, kwargs)
        finally:
            del frames
