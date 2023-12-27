"""blazingmq.dev.it.ito.proc


PURPOSE: Provide regular expressions that match log entries.
"""

import re

# Match log entries that are emitted by a node when the leader transitions to
# 'ACTIVE' status.  Capture the leader name.
leader = re.compile(r"new leader\: \[([^,]+), \d+\], leader status: ACTIVE")


def cluster_is_available(name):
    return re.compile(f"Cluster \\({name}\\) is available")
