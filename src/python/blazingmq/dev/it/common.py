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
blazingmq.dev.it.common

PURPOSE: Provide common types and functions for ITs.
"""

import traceback
from typing import Any, Dict, Optional


from blazingmq.core import BMQError


class BMQTestError(BMQError):
    """BlazingMQ test failure exception."""


def _context_to_str(context: Dict[str, Any]) -> str:
    context_str = "\n".join(f"    {key} = {val}" for key, val in context.items())
    if len(context_str) == 0:
        context_str = "EMPTY"
    return context_str


def BMQTST_ASSERT(condition, message: Optional[str] = None, **kwargs) -> None:
    if condition:
        return

    tb = traceback.StackSummary.extract(
        frame_gen=traceback.walk_stack(None), capture_locals=False
    )
    if isinstance(tb, list) and 2 <= len(tb):
        # [0] -> this function
        # [1] -> caller function
        fail_line = f"{tb[1].line} ({tb[1].filename}:{tb[1].lineno})"
    else:
        # Should never happen, don't want to use a nested assert to check it
        fail_line = "UNDEFINED"

    context_str = _context_to_str(kwargs)

    raise BMQTestError(
        f"{message or 'Failed condition'}\nFailure at '{fail_line}', context:\n{context_str}"
    )
