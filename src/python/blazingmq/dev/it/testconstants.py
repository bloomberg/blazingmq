DOMAIN_PRIORITY = "bmq.test.mmap.priority"
DOMAIN_PRIORITY_SC = "bmq.test.mmap.priority.sc"
DOMAIN_FANOUT = "bmq.test.mmap.fanout"
DOMAIN_FANOUT_SC = "bmq.test.mmap.fanout.sc"
DOMAIN_BROADCAST = "bmq.test.mem.broadcast"

TEST_QUEUE = "qqq"
TEST_APPIDS = ["foo", "bar", "baz"]

URI_PRIORITY = f"bmq://{DOMAIN_PRIORITY}/{TEST_QUEUE}"
URI_FANOUT = f"bmq://{DOMAIN_FANOUT}/{TEST_QUEUE}"
URI_FANOUT_FOO = f"bmq://{DOMAIN_FANOUT}/{TEST_QUEUE}?id=foo"
URI_FANOUT_BAR = f"bmq://{DOMAIN_FANOUT}/{TEST_QUEUE}?id=bar"
URI_FANOUT_BAZ = f"bmq://{DOMAIN_FANOUT}/{TEST_QUEUE}?id=baz"
URI_BROADCAST = f"bmq://{DOMAIN_BROADCAST}/{TEST_QUEUE}"

URI_PRIORITY_SC = f"bmq://{DOMAIN_PRIORITY_SC}/{TEST_QUEUE}"

URI_FANOUT_SC = f"bmq://{DOMAIN_FANOUT_SC}/{TEST_QUEUE}"
URI_FANOUT_SC_FOO = f"bmq://{DOMAIN_FANOUT_SC}/{TEST_QUEUE}?id=foo"
URI_FANOUT_SC_BAR = f"bmq://{DOMAIN_FANOUT_SC}/{TEST_QUEUE}?id=bar"
URI_FANOUT_SC_BAZ = f"bmq://{DOMAIN_FANOUT_SC}/{TEST_QUEUE}?id=baz"
